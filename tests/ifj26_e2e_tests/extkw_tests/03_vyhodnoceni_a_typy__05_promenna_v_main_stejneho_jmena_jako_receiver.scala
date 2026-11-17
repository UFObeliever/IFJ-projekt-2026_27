import ifj26.*

extension (value: Int)
  def twice: Int = value * 2

@main
def main(): Unit =
  val value = 7
  val r = value.twice
  print(r)
