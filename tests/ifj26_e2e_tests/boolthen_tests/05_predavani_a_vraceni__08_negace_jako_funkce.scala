import ifj26.*

def not(b: Boolean): Boolean = !b

@main
def main(): Unit =
  val r = not(true)
  val s = not(false)
  print(r)
  print(s)
