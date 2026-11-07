import ifj26.*

def fib(n: Int): Int =
  var a = 0
  var b = 1
  var i = 0
  while i < n do
    val t = a + b
    a = b
    b = t
    i = i + 1
  return a

@main def main(): Unit =
  val n = readInt()
  val r = fib(n)
  val s = toStr(r)
  print(s)
  print("\n")
