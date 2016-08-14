var fact, result;
fact = function(n,r) {
      if (n < 2){
         return r;
      }else return fact(n-1, r*n);
};
result = fact(100, 1);
