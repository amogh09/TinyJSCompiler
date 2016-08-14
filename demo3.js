var a = new Array(1);
var fib = function(x) {
            if (x > 1) {
              a[x] = fib(x-1) + fib(x-2);
              return fib(x-1) + fib(x-2);
            }
            else {
              a[x] = 1;
              return 1;
            }
          }
fib(7);
