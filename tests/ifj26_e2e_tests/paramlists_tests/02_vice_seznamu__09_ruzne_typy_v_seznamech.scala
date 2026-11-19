import ifj26.*

def rep(s: String)(n: Int): String =
  var r = ""
  var i = 0
  while i < n do
    r = r + s
    i = i + 1
  r

@main
def main(): Unit =
  val r = rep("ab")(3)
  print(r)
