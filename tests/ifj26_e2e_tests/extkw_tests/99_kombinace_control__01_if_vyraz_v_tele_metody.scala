import ifj26.*

extension (v: Int)
  def abs2: Int = if v < 0 then 0 - v else v

@main
def main(): Unit =
  val x = 0 - 7
  val r = x.abs2
  print(r)
