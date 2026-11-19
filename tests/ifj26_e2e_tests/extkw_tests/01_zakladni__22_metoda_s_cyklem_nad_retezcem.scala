import ifj26.*

extension (s: String)
  def rep(n: Int): String =
    var r = ""
    var i = 0
    while i < n do
      r = r + s
      i = i + 1
    r

@main
def main(): Unit =
  val a = "ab"
  val b = a.rep(3)
  print(b)
