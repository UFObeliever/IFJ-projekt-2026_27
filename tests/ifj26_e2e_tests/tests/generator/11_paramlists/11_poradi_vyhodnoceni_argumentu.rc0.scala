import ifj26.*

def say(n: Int): Int =
  print(n)
  n
def f(a: Int)(b: Int)(c: Int): Int = a + b + c

@main def main(): Unit =
  val x = f(say(1))(say(2))(say(3))
  print("\n")
  print(x)
  print("\n")
