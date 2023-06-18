
@.format_str = private unnamed_addr constant [5 x i8] c"%lf\0A\00", align 1

declare double @llvm.log.f64(double)
declare double @llvm.sqrt.f64(double)

define double @kl_math_sqrt(double %x) {
  %x.sqrt = call double @llvm.sqrt.f64(double %x)
  ret double %x.sqrt
}


define double @kl_math_log(double %x) {
  %x.log = call double @llvm.log.f64(double %x)
  ret double %x.log
}

define double @kl_std_max(double %x, double %y) {
  %cmp = fcmp olt double %x, %y
  %maxval = select i1 %cmp, double %y, double %x
  ret double %maxval
}

define double @kl_std_min(double %x, double %y) {
  %cmp = fcmp olt double %x, %y
  %minval = select i1 %cmp, double %x, double %y
  ret double %minval
}

define double @kl_putchard(double %x) {
  %chars_printed= tail call i32 (ptr, ...) @printf(ptr dereferenceable(1) @.format_str, double %x)
  %flag = icmp slt i32 %chars_printed, 0
  %retval = select i1 %flag, double 0.000000e+00, double 1.000000e+00
  ret double %retval
}


declare noundef i32 @printf(ptr readonly, ...) 

