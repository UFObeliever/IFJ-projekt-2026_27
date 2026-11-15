import ifj26.*

def ack(m: Int, n: Int): Int =
  if m == 0 then
    return n + 1
  else
    if n == 0 then
      val m1 = m - 1
      val r = ack(m1, 1)
      return r
    else
      val n1 = n - 1
      val m1 = m - 1
      val inner = ack(m, n1)
      val r = ack(m1, inner)
      return r

@main def main(): Unit =
  val a = ack(2, 3)
  print(a)
  print("\n")
