import ifj26.*

def add(a: Int)(b: Int)(c: Int): Int = a + b + c

@main
def main(): Unit =
  print(toStr(1 + add(1)(2)(3) * 2))
