import ifj26.*

def moves(n: Int): Int =
  if n == 0 then
    return 0
  else
    val m = n - 1
    val r = moves(m)
    return 2 * r + 1

@main def main(): Unit =
  val r = moves(10)
  print(r)
  print("\n")
