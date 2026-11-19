import ifj26.*

def f(a: Int)(): Int = a * 2

@main
def main(): Unit =
  val r = f(4)()
  print(r)
