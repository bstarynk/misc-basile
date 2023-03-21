// file misc-basile/ExBisonCpp/testbisoncp.yy
//
// this part is the BisonC++ directive
// See https://fbb-git.gitlab.io/bisoncpp

// ©2023 CEA and Basile Starynkevitch <basile@starynkevitch.net> and
// <basile.starynkevitch@cea.fr>

%thread-safe
%error-verbose
%baseclass-preinclude "testb.hh"
%class-header "_tb-parser.h"
%implementation-header "_tb-parsimpl.h"
%class-name "TbParser"
%debug
%polymorphic
%print-tokens
%% ///// this part has grammar rules

%start input

input: // empty
;

///// end of file misc-basile/ExBisonCpp/testbisoncp.yy