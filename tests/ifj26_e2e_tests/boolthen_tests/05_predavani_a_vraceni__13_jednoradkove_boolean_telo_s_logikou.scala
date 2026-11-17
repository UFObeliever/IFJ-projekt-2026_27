import ifj26.*

def between(x: Int, lo: Int, hi: Int): Boolean = x >= lo && x <= hi

@main
def main(): Unit =
  val a = between(5, 1, 10)
  val b = between(0, 1, 10)
  print(a)
  print(b)
