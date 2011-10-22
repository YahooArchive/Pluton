%module "Yahoo::pluton"
%{
#include <pluton/fault.h>
#include "swigClient.h"
#include "swigService.h"
%}
 
%include stl.i
%include <pluton/fault.h>
%include "swigClient.h"
%include "swigService.h"
