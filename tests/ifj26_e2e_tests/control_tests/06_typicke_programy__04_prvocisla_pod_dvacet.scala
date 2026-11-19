import ifj26.*

@main
def main(): Unit =
  for n <- 2 to 20 do
    var isPrime = true
    for d <- 2 until n do
      if n / d * d == n then
        isPrime = false
    if isPrime then
      print(n)
      print(" ")
