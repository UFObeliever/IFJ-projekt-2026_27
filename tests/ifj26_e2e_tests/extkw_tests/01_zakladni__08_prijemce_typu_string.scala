import ifj26.*

extension (s: String)
  def shout: String = s + "!"

@main
def main(): Unit =
  val a = "hey"
  val b = a.shout
  print(b)
