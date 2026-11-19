import ifj26.*

extension (s: String)
  def wrap(l: String, r: String): String = l + s + r

@main
def main(): Unit =
  val a = "x"
  val b = a.wrap("[", "]")
  print(b)
