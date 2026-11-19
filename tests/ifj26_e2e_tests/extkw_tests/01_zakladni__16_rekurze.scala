import ifj26.*

extension (n: Int)
  def fact: Int =
    if n <= 1 then
      return 1
    else
      val m = (n - 1).fact
      return n * m

@main
def main(): Unit =
  val x = 5
  val r = x.fact
  print(r)
