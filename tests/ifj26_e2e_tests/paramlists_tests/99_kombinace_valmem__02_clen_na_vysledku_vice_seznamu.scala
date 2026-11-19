import ifj26.*

def add(a: Int)(b: Int)(c: Int): Int = a + b + c

@main
def main(): Unit =
  val n = add(100)(200)(300).toString.length
  print(n)
