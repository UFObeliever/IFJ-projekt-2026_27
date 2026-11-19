import ifj26.*

extension (s: String)
  def wordCount: Int =
    var n = 0
    if strLength(s) > 0 then
      n = 1
    n

@main
def main(): Unit =
  val a = "x"
  val b = ""
  val r1 = a.wordCount
  val r2 = b.wordCount
  print(r1)
  print(r2)
