import ifj26.*

extension (v: Int)
  // první
  def twice: Int = v * 2
  /* druhá */
  def thrice: Int = v * 3

@main
def main(): Unit =
  val x = 1
  val a = x.twice
  val b = x.thrice
  print(a)
  print(b)
