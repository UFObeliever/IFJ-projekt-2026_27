import ifj26.*

def and3(a: Boolean, b: Boolean, c: Boolean): Boolean = a && b && c

@main
def main(): Unit =
  val r = and3(true, true, false)
  val s = and3(true, true, true)
  print(r)
  print(s)
