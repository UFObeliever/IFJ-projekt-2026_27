import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

extension (s: String)
  def twice: String = s + s

@main
def main(): Unit =
  val x = 12
  val a = x.twice
  val b = "ab".twice
  print(a)
  print(b)
