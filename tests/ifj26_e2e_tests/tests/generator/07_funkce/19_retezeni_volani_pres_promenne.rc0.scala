import ifj26.*

def f(x: Int): Int = x + 1
def g(x: Int): Int = x * 2
def h(x: Int): Int = x - 3

@main def main(): Unit =
  val a = f(2)
  val b = g(a)
  val c = h(b)
  print(c)
  print("\n")
