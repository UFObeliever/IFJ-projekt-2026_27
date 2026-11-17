import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  val x = 4
  val r = if x > 3 then x.twice else x
  print(r)
