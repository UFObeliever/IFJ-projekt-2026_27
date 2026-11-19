import ifj26.*

def join(a: String, b: String): String = a + b

@main
def main(): Unit =
  print(join("x" + "y", toStr(2 * 2)))
