import ifj26.*

extension (s: String)
  def shout: String = s + "!"

@main
def main(): Unit =
  val n = 5
  val m = s"n=$n".shout
  print(m)
