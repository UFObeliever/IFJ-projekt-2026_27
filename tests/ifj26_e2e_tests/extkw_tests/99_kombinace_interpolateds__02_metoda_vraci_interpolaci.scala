import ifj26.*

extension (n: Int)
  def tag: String = s"<$n>"

@main
def main(): Unit =
  val x = 5
  val r = x.tag
  print(r)
