import ifj26.*

def add(a: Int)(b: Int)(c: Int): Int = a + b + c

@main
def main(): Unit =
  val r = add(1)(2)(3)
  print(r)
