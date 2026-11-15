import ifj26.*

@main def main(): Unit =
  var n = 2
  var count = 0
  while n < 100 do
    var d = 2
    var isPrime = 1
    while d * d <= n do
      val r = n - n / d * d
      if r == 0 then
        isPrime = 0
      else
        isPrime = isPrime
      d = d + 1
    if isPrime == 1 then
      count = count + 1
    else
      count = count
    n = n + 1
  print(count)
  print("\n")
