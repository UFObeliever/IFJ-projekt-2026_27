import ifj26.*

@main def main(): Unit =
  var n = 2
  while n < 500 do
    var sum = 0
    var d = 1
    while d < n do
      val r = n - n / d * d
      if r == 0 then
        sum = sum + d
      else
        sum = sum
      d = d + 1
    if sum == n then
      print(n)
      print(" ")
    else
      sum = sum
    n = n + 1
  print("\n")
