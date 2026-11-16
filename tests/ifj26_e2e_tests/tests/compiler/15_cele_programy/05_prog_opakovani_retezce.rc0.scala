import ifj26.*

def repeat(s: String, n: Int): String =
  var result = ""
  var i = 0
  while i < n do
    result = result + s
    i = i + 1
  return result

@main def main(): Unit =
  val r = repeat("ab", 3)
  print(r)
