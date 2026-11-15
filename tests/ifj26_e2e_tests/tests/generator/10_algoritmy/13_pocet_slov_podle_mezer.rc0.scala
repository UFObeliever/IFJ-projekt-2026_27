import ifj26.*

@main def main(): Unit =
  val s = "the quick brown fox"
  val n = strLength(s)
  var i = 0
  var words = 1
  while i < n do
    val c = ord(s, i)
    if c == 32 then
      words = words + 1
    else
      words = words
    i = i + 1
  print(words)
  print("\n")
