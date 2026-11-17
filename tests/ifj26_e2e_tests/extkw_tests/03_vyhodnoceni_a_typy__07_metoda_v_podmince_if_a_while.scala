import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  val x = 3
  if x.twice > 5 then
    print("big")
  else
    print("small")
  var i = 0
  while i.twice < 6 do
    i = i + 1
  print(i)
