import ifj26.*

def f(a: Int)(b: Int): Int = a * 10 + b

@main
def main(): Unit =
  val r = f(3)(4)
  print(r)
