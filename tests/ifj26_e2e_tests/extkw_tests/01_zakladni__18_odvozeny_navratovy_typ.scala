import ifj26.*

extension (value: Int)
  def triple = value * 3

@main
def main(): Unit =
  val x = 4
  val r = x.triple
  print(r)
