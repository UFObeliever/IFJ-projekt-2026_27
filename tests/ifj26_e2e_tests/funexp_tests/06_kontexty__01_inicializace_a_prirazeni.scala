import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  val a = dbl(3) + 1
  var b = dbl(a) - 2
  b = b + dbl(b)
  print(toStr(a) + " " + toStr(b))
