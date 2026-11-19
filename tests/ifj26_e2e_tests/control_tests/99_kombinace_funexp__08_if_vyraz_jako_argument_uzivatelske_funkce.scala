import ifj26.*

def add(a: Int, b: Int): Int = a + b

@main
def main(): Unit =
  val c = true
  print(toStr(add(if c then 1 else 2, 3)))
