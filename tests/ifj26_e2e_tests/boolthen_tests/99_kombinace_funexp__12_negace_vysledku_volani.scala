import ifj26.*

def isPos(n: Int): Boolean = n > 0

@main
def main(): Unit =
  val r = !isPos(5)
  print(r)
