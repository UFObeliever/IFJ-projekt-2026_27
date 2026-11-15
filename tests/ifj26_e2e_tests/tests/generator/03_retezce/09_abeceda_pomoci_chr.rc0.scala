import ifj26.*

@main def main(): Unit =
  var s = ""
  var i = 0
  while i < 26 do
    val code = 65 + i
    val ch = chr(code)
    s = s + ch
    i = i + 1
  print(s)
  print("\n")
