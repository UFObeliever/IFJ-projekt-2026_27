import ifj26.*

def add(a: Int)(b: Int)(c: Int): Int = a + b + c

@main
def main(): Unit =
  print(toStr(add(add(1)(2)(3))(4)(5)))
