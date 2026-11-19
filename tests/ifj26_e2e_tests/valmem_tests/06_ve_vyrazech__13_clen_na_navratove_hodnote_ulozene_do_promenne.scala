import ifj26.*

def twice(x: Int): Int = x * 2

@main
def main(): Unit =
  val r = twice(21)
  val s = r.toString
  print(s)
