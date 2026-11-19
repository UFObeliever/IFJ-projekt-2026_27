import ifj26.*

@main
def main(): Unit =
  var x = 4
  val absolute = if x < 0 then 0 - x else x
  print(absolute)
