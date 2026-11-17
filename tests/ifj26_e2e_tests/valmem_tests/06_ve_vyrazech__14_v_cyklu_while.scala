import ifj26.*

@main
def main(): Unit =
  val s = "xyz"
  var i = 0
  var total = 0
  while i < 3 do
    total = total + s.length
    i = i + 1
  print(total)
