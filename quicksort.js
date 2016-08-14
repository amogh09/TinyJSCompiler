//Quicksort.js
var arr = [3, 1, 5, 3, 5, 6, 7, 2, 10, 23, 34, 23, 1, 2, 0]

var sort = function(arr, l, r){
  if(l < r){
    var pivot = arr[r]
    var wall = l
    for(var i=l; i<r; i++){
      if(arr[i] < pivot){
        swap(arr, wall, i)
        wall++
      }
    }
    swap(arr, wall, r)
    sort(arr, l, wall-1)
    sort(arr, wall+1, r)
  }
}

var swap = function(arr, a, b){
  var tmp = arr[a]
  arr[a] = arr[b]
  arr[b] = tmp
}

sort(arr, 0, 14)
arr[3]
