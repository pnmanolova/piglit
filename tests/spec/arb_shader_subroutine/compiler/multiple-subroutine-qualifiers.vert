// [config]
// expect_result: fail
// glsl_version: 1.50
// require_extensions: GL_ARB_shader_subroutine
// [end config]
//
// Test for compile error when there are multiple subroutine qualifiers for a
// single function.

#version 150
#extension GL_ARB_shader_subroutine: require

subroutine void func_type();

/* A subroutine matching the above type */
subroutine (func_type) subroutine (func_type) void f() {}
