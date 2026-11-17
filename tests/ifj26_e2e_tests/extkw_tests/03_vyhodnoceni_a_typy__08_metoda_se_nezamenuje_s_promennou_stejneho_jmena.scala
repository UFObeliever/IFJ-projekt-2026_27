import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  val twice = 99
  val x = 5
  val r = x.twice
  print(r)
  print(",")
  print(twice)
