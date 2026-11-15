import ifj26.*

@main def main(): Unit =
  val s = "hello"
  val n = strLength(s)
  var i = 0
  var r = ""
  while i < n do
    val c = ord(s, i)
    val up = c - 32
    val ch = chr(up)
    r = r + ch
    i = i + 1
  print(r)
  print("\n")
