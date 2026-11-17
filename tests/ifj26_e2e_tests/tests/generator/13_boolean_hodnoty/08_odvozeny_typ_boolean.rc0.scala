import ifj26.*

def inRange(x: Int, lo: Int, hi: Int) = lo <= x == x <= hi

@main def main(): Unit =
  val a = inRange(5, 1, 10)
  val b = inRange(0, 1, 10)
  print(a)
  print("\n")
  print(b)
  print("\n")
