import ifj26.*

@main def main(): Unit =
  val s = "stressed"
  val n = strLength(s)
  var i = n - 1
  var r = ""
  while i >= 0 do
    val j = i + 1
    val ch = subStr(s, i, j)
    r = r + ch
    i = i - 1
  print(r)
  print("\n")
