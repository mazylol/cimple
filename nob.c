#include <stdlib.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"
#define BINARY_NAME "cimple"
#define INSTALL_PATH "/usr/local/bin/" BINARY_NAME

int create_database(const char *sources[], size_t sources_count,
                    const char *cflags[], size_t cflags_count,
                    const char *cc) {

    Nob_String_Builder sb = {0};
    nob_sb_append_cstr(&sb, "[\n");

    char *cwd = getcwd(NULL, 0);
    if (!cwd)
        return 1;

    for (size_t i = 0; i < sources_count; ++i) {
        const char *src = sources[i];
        char *abs_src = realpath(src, NULL);

        nob_sb_append_cstr(&sb, "  {\n");
        nob_sb_append_cstr(&sb, "    \"directory\": \"");
        nob_sb_append_cstr(&sb, cwd);
        nob_sb_append_cstr(&sb, "\",\n");

        nob_sb_append_cstr(&sb, "    \"file\": \"");
        nob_sb_append_cstr(&sb, abs_src ? abs_src : src);
        nob_sb_append_cstr(&sb, "\",\n");

        nob_sb_append_cstr(&sb, "    \"arguments\": [\"");
        nob_sb_append_cstr(&sb, cc);
        nob_sb_append_cstr(&sb, "\", \"-c\", \"");
        nob_sb_append_cstr(&sb, src);
        nob_sb_append_cstr(&sb, "\"");

        for (size_t j = 0; j < cflags_count; ++j) {
            nob_sb_append_cstr(&sb, ", \"");
            nob_sb_append_cstr(&sb, cflags[j]);
            nob_sb_append_cstr(&sb, "\"");
        }

        nob_sb_append_cstr(&sb, "]\n");

        if (i < sources_count - 1) {
            nob_sb_append_cstr(&sb, "  },\n");
        } else {
            nob_sb_append_cstr(&sb, "  }\n");
        }
        free(abs_src);
    }
    nob_sb_append_cstr(&sb, "]\n");

    nob_write_entire_file(BUILD_FOLDER "compile_commands.json", sb.items, sb.count);
    nob_sb_free(sb);
    free(cwd);

    return 0;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    // 1. Check if we are installing early
    bool is_install = (argc > 1 && strcmp(argv[1], "install") == 0);

    const char *cc = "clang";

    // 2. Define flags based on mode
    // Note: We remove -O0 and -g for install, and add -O3
    const char *common_flags[] = {"-fdiagnostics-color=always", "-D_FILE_OFFSET_BITS=64", "-Wall", "-Wextra", "-std=c99"};

    Nob_Cmd cflags = {0};

    for (size_t i = 0; i < NOB_ARRAY_LEN(common_flags); ++i) {
        nob_da_append(&cflags, common_flags[i]);
    }

    if (is_install) {
        nob_da_append(&cflags, "-O3");
    } else {
        nob_da_append(&cflags, "-O0");
        nob_da_append(&cflags, "-g");
    }

    const char *libs[] = {};
    const char *sources[] = {
        SRC_FOLDER "main.c",
        SRC_FOLDER "lexer.c",
        SRC_FOLDER "dynamic.c"};

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER))
        return 1;

    // Generate database with the current flags
    if (create_database(sources, NOB_ARRAY_LEN(sources), cflags.items, cflags.count, cc) != 0) {
        return 1;
    }

    Nob_Cmd cmd = {0};
    Nob_File_Paths object_files = {0};
    Nob_Procs procs = {0};

    // 3. Compile Step
    for (size_t i = 0; i < NOB_ARRAY_LEN(sources); ++i) {
        const char *src_path = sources[i];
        const char *obj_path = nob_temp_sprintf("%s%s.o", BUILD_FOLDER, nob_path_name(src_path));
        nob_da_append(&object_files, obj_path);

        // NOTE: If you switch from dev to install, you usually want to force a rebuild
        // because the flags changed, but the file timestamps might not have.
        if (is_install || nob_needs_rebuild1(obj_path, src_path)) {
            cmd.count = 0;
            nob_cmd_append(&cmd, cc);
            nob_cmd_append(&cmd, "-c", src_path);
            nob_cmd_append(&cmd, "-o", obj_path);
            nob_da_append_many(&cmd, cflags.items, cflags.count);

            if (!nob_cmd_run(&cmd, .async = &procs))
                return 1;
        }
    }

    if (!nob_procs_wait(procs))
        return 1;

    // 4. Link Step
    const char *binary_path = BUILD_FOLDER BINARY_NAME;
    if (is_install || nob_needs_rebuild(binary_path, object_files.items, object_files.count)) {
        cmd.count = 0;
        nob_cmd_append(&cmd, cc);
        nob_cmd_append(&cmd, "-o", binary_path);
        nob_da_append_many(&cmd, object_files.items, object_files.count);
        nob_da_append_many(&cmd, libs, NOB_ARRAY_LEN(libs));
        if (!nob_cmd_run(&cmd))
            return 1;
    }

    // 5. Install Step
    if (is_install) {
        nob_log(NOB_INFO, "Installing optimized binary to %s...", INSTALL_PATH);
        if (!nob_copy_file(binary_path, INSTALL_PATH)) {
            nob_log(NOB_ERROR, "Installation failed. Do you need 'sudo'?");
            return 1;
        }
        nob_log(NOB_INFO, "Successfully installed!");
    }

    // Cleanup
    nob_cmd_free(cmd);
    nob_da_free(object_files);
    nob_da_free(procs);
    // Note: cflags_builder.items is used as cflags, so free it last
    nob_cmd_free(cflags);

    return 0;
}
