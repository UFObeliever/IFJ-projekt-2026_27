import ifj26.*

extension (s: String)
  def shout: String = s + "!"

@main
def main(): Unit =
  val b = "hey".shout
  print(b)
