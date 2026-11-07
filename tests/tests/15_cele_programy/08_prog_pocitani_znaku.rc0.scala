import ifj26.*

def countA(s: String): Int =
  var i = 0
  var c = 0
  val n = strLength(s)
  while i < n do
    val ch = ord(s, i)
    if ch == 97 then
      c = c + 1
    else
      ()
    i = i + 1
  return c

@main def main(): Unit =
  val s = readLine()
  val c = countA(s)
  print(c)
