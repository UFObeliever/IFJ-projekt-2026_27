import ifj26.*

def even(n: Int): Boolean = n / 2 * 2 == n

@main def main(): Unit =
  val e = even(4)
  print(e)
