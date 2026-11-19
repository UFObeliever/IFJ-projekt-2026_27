import ifj26.*

extension (s: String)
  def len2: Int = strLength(s) * 2

@main
def main(): Unit =
  val a = "abc"
  print(toStr(a.len2))
