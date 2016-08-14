var a,b,y;
a = function(x) {
      var z;
      y = function() {
            return z + x;
          }
      z = x * x;
      return y();
    }
b = a(2);
