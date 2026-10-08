// Copyright Seong Woo Lee. All Rights Reserved.

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "third_party/llvm/include/clang-c/Index.h"
#include "third_party/llvm/include/clang-c/CXCompilationDatabase.h"

#define ArrayCount(_) (sizeof(_)/sizeof(_[0]))

#define MAX_ARGS 512

static bool
starts_with(const char *s, const char *prefix)
{
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

static bool
ends_with(const char *s, const char *suffix)
{
  size_t n = strlen(s);
  size_t m = strlen(suffix);
  return (n >= m) && (strcmp(s + n - m, suffix) == 0);
}

// Matches annotations made by meta/meta.h, e.g. FIELD() -> "FIELD", FIELD(foo) -> "FIELDfoo".
static bool
has_annotation(CXCursor cursor, const char *annotation)
{
  struct Query {
    bool found;
    const char *annotation;
  } q;

  q.found = false;
  q.annotation = annotation;

  clang_visitChildren(cursor, [](CXCursor c, CXCursor p, CXClientData d) {
    if (clang_getCursorKind(c) == CXCursor_AnnotateAttr)
    {
      Query *q = (Query*)d;
      CXString s = clang_getCursorSpelling(c);
      q->found = starts_with(clang_getCString(s), q->annotation);
      clang_disposeString(s);
      if (q->found) return CXChildVisit_Break;
    }
    return CXChildVisit_Continue;
  },
  &q);
  return q.found;
}

static CXChildVisitResult
clang_visitor(CXCursor     cursor,
              CXCursor     parent,
              CXClientData client_data)
{
  if (!clang_Location_isFromMainFile(clang_getCursorLocation(cursor)))
  {
    return CXChildVisit_Continue;
  }

  CXCursorKind kind = clang_getCursorKind(cursor);

  if (kind == CXCursor_Namespace)
  {
    return CXChildVisit_Recurse;
  }

  if ((kind == CXCursor_StructDecl) || (kind == CXCursor_ClassDecl))
  {
    if (clang_isCursorDefinition(cursor) && has_annotation(cursor, "REFLECT"))
    {
      CXString struct_name = clang_getCursorSpelling(cursor);
      printf("struct %s, size: %lld, align: %lld\n",
             clang_getCString(struct_name),
             clang_Type_getSizeOf(clang_getCursorType(cursor)),
             clang_Type_getAlignOf(clang_getCursorType(cursor)));
      clang_disposeString(struct_name);

      clang_visitChildren(cursor, [](CXCursor c, CXCursor p, CXClientData d) {
        if (clang_getCursorKind(c) == CXCursor_FieldDecl && has_annotation(c, "FIELD"))
        {
          CXType field_type = clang_getCursorType(c);
          CXString name     = clang_getCursorSpelling(c);
          CXString type     = clang_getTypeSpelling(field_type);
          long long offset  = clang_Type_getOffsetOf(clang_getCursorType(p), clang_getCString(name));
          long long size    = clang_Type_getSizeOf(field_type);
          long long align   = clang_Type_getAlignOf(field_type);

          if (offset < 0 || size < 0 || align < 0)
          {
            printf("  %s %s <layout error, offset: %lld, size: %lld, align: %lld>\n",
                   clang_getCString(type), clang_getCString(name), offset, size, align);
          }
          else
          {
            printf("  %s %s, offset: %lld, size: %lld, align: %lld\n",
                   clang_getCString(type), clang_getCString(name), offset / 8, size, align);
          }

          clang_disposeString(type);
          clang_disposeString(name);
        }
        return CXChildVisit_Continue;
      },
      NULL);
    }
  }

  return CXChildVisit_Continue;
}

// Fills `args` with the engine's compile command (args[0] = cl.exe) taken from
// compile_commands.json in `build_dir`, minus flags that only matter for a real compile.
static int
engine_args_from_compile_db(const char *build_dir, char **args, int max_args)
{
  CXCompilationDatabase_Error err;
  CXCompilationDatabase db = clang_CompilationDatabase_fromDirectory(build_dir, &err);
  if (err != CXCompilationDatabase_NoError)
  {
    printf("Failed to load compile_commands.json from: %s\n", build_dir);
    return 0;
  }

  // Any rts_engine C++ source shares the engine's options (defines, includes, std, forced pch.h).
  CXCompileCommands cmds = clang_CompilationDatabase_getAllCompileCommands(db);
  CXCompileCommand  cmd  = NULL;
  for (unsigned i = 0; i < clang_CompileCommands_getSize(cmds) && !cmd; i += 1)
  {
    CXCompileCommand candidate = clang_CompileCommands_getCommand(cmds, i);
    CXString file = clang_CompileCommand_getFilename(candidate);
    bool is_cpp = ends_with(clang_getCString(file), ".cpp");
    clang_disposeString(file);
    if (!is_cpp) continue;

    unsigned n = clang_CompileCommand_getNumArgs(candidate);
    for (unsigned j = 0; j < n; j += 1)
    {
      CXString a = clang_CompileCommand_getArg(candidate, j);
      if (starts_with(clang_getCString(a), "/Fo") && strstr(clang_getCString(a), "rts_engine.dir"))
      {
        cmd = candidate;
      }
      clang_disposeString(a);
    }
  }

  int num_args = 0;
  if (cmd)
  {
    unsigned n = clang_CompileCommand_getNumArgs(cmd);
    for (unsigned i = 0; i < n && num_args < max_args; i += 1)
    {
      CXString s = clang_CompileCommand_getArg(cmd, i);
      const char *a = clang_getCString(s);

      bool skip =
        starts_with(a, "/Yu") || starts_with(a, "/Yc") || starts_with(a, "/Fp") || // MSVC pch is unusable by clang.
        starts_with(a, "/Fo") || starts_with(a, "/Fd") ||                          // Outputs.
        (strcmp(a, "/FS") == 0) || (strcmp(a, "/WX") == 0) ||
        (strcmp(a, "-c") == 0)  || (strcmp(a, "/c") == 0) || (strcmp(a, "--") == 0) ||
        (i == n - 1);                                                              // The source file itself.

      if (!skip)
      {
        args[num_args++] = _strdup(a);
      }
      clang_disposeString(s);
    }
  }
  else
  {
    printf("No rts_engine C++ source found in compile_commands.json from: %s\n", build_dir);
  }

  clang_CompileCommands_dispose(cmds);
  clang_CompilationDatabase_dispose(db);
  return num_args;
}

int main(int argc, char **argv)
{
  if (argc < 3) {
    printf("Metaprogramming\n");
    printf("Usage: meta <build_dir> <files...>\n");
    printf("  build_dir: CMake build directory with compile_commands.json, e.g. build/debug\n");
    return -1;
  }

  const char *build_dir = argv[1];

  char *cli_args[MAX_ARGS];
  int num_cli_args = engine_args_from_compile_db(build_dir, cli_args, MAX_ARGS - 4);
  if (num_cli_args == 0)
  {
    return -1;
  }

  // args[0] is cl.exe; make the clang-cl driver mode explicit rather than relying on the program name.
  for (int i = num_cli_args; i > 1; i -= 1)
  {
    cli_args[i] = cli_args[i-1];
  }
  cli_args[1] = _strdup("--driver-mode=cl");
  num_cli_args += 1;

  cli_args[num_cli_args++] = _strdup("/TP");           // Treat headers as C++ as well.
  cli_args[num_cli_args++] = _strdup("-DBUILD_META=1");


  int result = 0;
  CXIndex index = clang_createIndex(0, 1);

  // Iterate over files.
  for (int i = 2; i < argc; i += 1)
  {
    char *file_path = argv[i];

    printf("Inspecting %s\n", file_path);

    CXTranslationUnit tu = NULL;
    CXErrorCode error = clang_parseTranslationUnit2FullArgv(index,
                                                            file_path,
                                                            cli_args,
                                                            num_cli_args,
                                                            NULL, 0,
                                                            CXTranslationUnit_SkipFunctionBodies,
                                                            &tu);
    if (error != CXError_Success || !tu)
    {
      printf("Failed to create translation unit from file: %s (error %d)\n", file_path, error);
      result = -1;
      continue;
    }

    bool has_errors = false;
    for (unsigned d = 0; d < clang_getNumDiagnostics(tu); d += 1)
    {
      CXDiagnostic diag = clang_getDiagnostic(tu, d);
      if (clang_getDiagnosticSeverity(diag) >= CXDiagnostic_Error)
      {
        has_errors = true;
      }
      clang_disposeDiagnostic(diag);
    }

    if (has_errors)
    {
      printf("Errors while parsing %s, skipping.\n", file_path);
      result = -1;
    }
    else
    {
      clang_visitChildren(clang_getTranslationUnitCursor(tu), clang_visitor, NULL);
    }

    clang_disposeTranslationUnit(tu);
  }

  clang_disposeIndex(index);

  for (int i = 0; i < num_cli_args; i += 1)
  {
    free(cli_args[i]);
  }

  return result;
}
