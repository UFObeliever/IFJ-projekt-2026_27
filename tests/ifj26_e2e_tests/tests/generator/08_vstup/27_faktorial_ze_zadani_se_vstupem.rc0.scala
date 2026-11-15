import ifj26.*

def factorial(n: Int): Int =
  var result = 1
  var i: Int = n
  while i > 1 do
    result = result * i
    i = i - 1
  result

@main def runFoo(): Unit =
  val n = readInt()
  val result = factorial(n)
  print(result)
  print("\n")
