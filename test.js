var arr = [5, 2, 3, 1, 4];

var sort = function(arr, l, r){
  if(l >= r)
    return;

  var wall = l;
  var pivot = arr[r];

  for(var i=l; i<r; i++){
    if(arr[i] < pivot){
      swap(arr, wall, i);
      wall++;
    }
  }
  swap(arr, wall, r);
  sort(arr, l, wall-1);
  sort(arr, wall+1, r);
}

var swap = function(arr, a, b){
  var tmp = arr[a];
  arr[a] = arr[b];
  arr[b] = tmp;
}

sort(arr, 0, 4);

arr[0]
