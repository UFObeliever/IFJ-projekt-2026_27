import ifj26.*

extension (s: String)
  def shout: String = s + "!"

@main
def main(): Unit =
  val n = "abc".shout.length
  print(n)
